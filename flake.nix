{
    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    };

    outputs = { self, nixpkgs, ... }: let
        systems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
    in {
        devShells = nixpkgs.lib.genAttrs systems (system: let
            pkgs = nixpkgs.legacyPackages.${system};
        in {
            default = pkgs.mkShell {
                name = "hurdygurdy";

                packages = with pkgs; [
                    gnumake
                    shaderc
                    sdl3
                    vulkan-validation-layers
                ];

                LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath [
                    pkgs.vulkan-loader
                ];
            };
        });
    };
}
