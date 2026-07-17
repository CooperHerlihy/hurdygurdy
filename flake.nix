{
    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
        # for Vulkan validation layers, because latest version is broken
        nixpkgs-vvl.url = "github:nixos/nixpkgs/49928bd4ad0aa8fb021c1ac71e8dc5921abf9783";
    };

    outputs = { self, nixpkgs, nixpkgs-vvl, ... }: let
        systems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
        forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f system);
    in {
        devShells = forAllSystems (system: let
            pkgs = nixpkgs.legacyPackages.${system};
            pkgs-vvl = nixpkgs-vvl.legacyPackages.${system};
        in {
            default = pkgs.mkShell {
                name = "hurdygurdy";

                packages = with pkgs; [
                    cmake
                    ninja

                    clang
                    mold
                    ccache

                    shaderc
                    pkgs-vvl.vulkan-validation-layers

                    gdb
                    sdl3
                    vulkan-headers
                ];

                buildInputs = with pkgs; [
                    sdl3
                    vulkan-headers
                ];

                LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath [
                    pkgs.vulkan-loader
                ];
            };
        });

        packages = forAllSystems (system: let
            pkgs = nixpkgs.legacyPackages.${system};

            imgui-src = pkgs.fetchFromGitHub {
                owner = "ocornut";
                repo = "imgui";
                rev = "2af6dd9694288e6befe1edb7ce25510911693c22";
                hash = "sha256-ocCgBM2uHDhdur81VKuKJNoa0TEvhfjhjfJlycC5YpI=";
            };

        in {
            default = pkgs.stdenv.mkDerivation {
                name = "hurdygurdy";
                src = self;

                nativeBuildInputs = with pkgs; [
                    cmake
                    ninja

                    clang
                    mold
                    ccache

                    shaderc
                ];

                buildInputs = with pkgs; [
                    sdl3
                    vulkan-headers
                ];

                cmakeBuildType = "Release";

                preConfigure = ''
                    rm -rf vendor/imgui
                    cp -r ${imgui-src} vendor/imgui
                    chmod -R u+w vendor/imgui
                '';

                postFixup = ''
                    for bin in $out/bin/*; do
                        patchelf --add-rpath ${pkgs.vulkan-loader}/lib $bin
                    done
                '';
            };
        });
    };
}
