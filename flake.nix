{
    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    };

    outputs = { self, nixpkgs, ... }: let
        systems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
        forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f system);
    in {
        devShells = forAllSystems (system: let
            pkgs = nixpkgs.legacyPackages.${system};

            debug = pkgs.writeShellScriptBin "debug" "cmake --workflow --preset debug && ./build/tests";
            release = pkgs.writeShellScriptBin "release" "cmake --workflow --preset release && ./build/release/tests";
            san = pkgs.writeShellScriptBin "san" "cmake --workflow --preset san && LSAN_OPTIONS=detect_leaks=0 ./build/san/tests";
            tsan = pkgs.writeShellScriptBin "tsan" "cmake --workflow --preset tsan && TSAN_OPTIONS=suppressions=/dev/null ./build/tsan/tests";
        in {
            default = pkgs.mkShell.override {
                stdenv = pkgs.clang19Stdenv;
            } {
                name = "hurdygurdy";

                packages = with pkgs; [
                    clang-tools
                    cmake
                    ninja
                    mold
                    ccache

                    shaderc
                    vulkan-validation-layers

                    gdb
                    valgrind
                    renderdoc
                    perf

                    debug
                    release
                    san
                    tsan
                ];

                LD_LIBRARY_PATH = with pkgs; lib.makeLibraryPath [
                    vulkan-loader
                    sdl3  # fallback
                    libx11
                    libxrandr
                    pipewire
                    libxkbcommon
                    libevdev
                ];
            };
        });

        packages = forAllSystems (system: let
            pkgs = nixpkgs.legacyPackages.${system};

            vulkan-headers-src = pkgs.fetchFromGitHub {
                owner = "KhronosGroup";
                repo = "Vulkan-Headers";
                rev = "e3b1eec08173d6b825cd3ac88c885a63b621504a";
                hash = "sha256-tAYvYx/Mqvf/I177xmx7oLZVc7S7GK3MArY3i+FCYuw=";
            };

            imgui-src = pkgs.fetchFromGitHub {
                owner = "ocornut";
                repo = "imgui";
                rev = "2af6dd9694288e6befe1edb7ce25510911693c22";
                hash = "sha256-ocCgBM2uHDhdur81VKuKJNoa0TEvhfjhjfJlycC5YpI=";
            };

            sdl3-src = pkgs.fetchFromGitHub {
                owner = "libsdl-org";
                repo = "SDL";
                rev = "release-3.4.10";
                hash = "sha256-6Dph2eLiJUmpQzPWe8EuY5LrWhrFwde2f2dwfgCcWNw=";
            };

            xorgproto-src = pkgs.fetchFromGitLab {
                domain = "gitlab.freedesktop.org";
                owner = "xorg";
                repo = "proto";
                rev = "xorgproto-2024.1";
                hash = "sha256-1O9XR2V0lR4Rh8FAVGV4qOYfOXC/n+8sUyM5l4pFRw4=";
            };

            libX11-src = pkgs.fetchFromGitLab {
                domain = "gitlab.freedesktop.org";
                owner = "xorg";
                repo = "lib";
                rev = "libX11-1.8.10";
                hash = "sha256-I0tLlG/gxF+2dV4q1Q9tFJq3a6g8x3J5l5J5l5J5l5=";
            };

            libXrandr-src = pkgs.fetchFromGitLab {
                domain = "gitlab.freedesktop.org";
                owner = "xorg";
                repo = "lib";
                rev = "libXrandr-1.5.4";
                hash = "sha256-PLACEHOLDER";
            };

            libXrender-src = pkgs.fetchFromGitLab {
                domain = "gitlab.freedesktop.org";
                owner = "xorg";
                repo = "lib";
                rev = "libXrender-0.9.10";
                hash = "sha256-PLACEHOLDER";
            };

            pipewire-src = pkgs.fetchFromGitLab {
                domain = "gitlab.freedesktop.org";
                owner = "pipewire";
                repo = "pipewire";
                rev = "1.0.7";
                hash = "sha256-1J5J5J5J5J5J5J5J5J5J5J5J5J5J5J5J5J5J5J5J5=";
            };

            libxkbcommon-src = pkgs.fetchFromGitHub {
                owner = "xkbcommon";
                repo = "libxkbcommon";
                rev = "xkbcommon-1.6.0";
                hash = "sha256-1K5K5K5K5K5K5K5K5K5K5K5K5K5K5K5K5K5K5K5K5=";
            };

        in {
            default = pkgs.clang19Stdenv.mkDerivation {
                name = "hurdygurdy";
                src = self;

                nativeBuildInputs = with pkgs; [
                    clang-tools
                    cmake
                    ninja
                    mold
                    shaderc
                ];

                cmakeBuildType = "Release";

                preConfigure = ''
                    rm -rf vendor/imgui vendor/Vulkan-Headers vendor/SDL
                    rm -rf vendor/xorgproto vendor/libX11 vendor/libXrandr vendor/libXrender
                    rm -rf vendor/pipewire vendor/libxkbcommon
                    cp -r ${vulkan-headers-src} vendor/Vulkan-Headers
                    cp -r ${imgui-src} vendor/imgui
                    cp -r ${sdl3-src} vendor/SDL
                    cp -r ${xorgproto-src} vendor/xorgproto
                    cp -r ${libX11-src} vendor/libX11
                    cp -r ${libXrandr-src} vendor/libXrandr
                    cp -r ${libXrender-src} vendor/libXrender
                    cp -r ${pipewire-src} vendor/pipewire
                    cp -r ${libxkbcommon-src} vendor/libxkbcommon
                    chmod -R u+w vendor/Vulkan-Headers
                    chmod -R u+w vendor/imgui
                    chmod -R u+w vendor/SDL
                    chmod -R u+w vendor/xorgproto
                    chmod -R u+w vendor/libX11
                    chmod -R u+w vendor/libXrandr
                    chmod -R u+w vendor/libXrender
                    chmod -R u+w vendor/pipewire
                    chmod -R u+w vendor/libxkbcommon
                '';

                postFixup = ''
                    for bin in $out/bin/*; do
                        patchelf --add-rpath ${pkgs.vulkan-loader}/lib $bin
                        patchelf --add-rpath ${pkgs.sdl3}/lib $bin  # fallback
                        patchelf --add-rpath ${pkgs.libx11}/lib $bin
                        patchelf --add-rpath ${pkgs.libxrandr}/lib $bin
                        patchelf --add-rpath ${pkgs.pipewire}/lib $bin
                        patchelf --add-rpath ${pkgs.libxkbcommon}/lib $bin
                        patchelf --add-rpath ${pkgs.libevdev}/lib $bin
                    done
                '';
            };
        });
    };
}
