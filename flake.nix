{
    description = "HurdyGurdy Development Environment";

    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    };

    outputs = { self, nixpkgs, ... }:
    let
        system = "x86_64-linux";
        pkgs = import nixpkgs { inherit system; };
    in {
        devShells."${system}".default = pkgs.mkShell {
            name = "hurdygurdy";
            shellHook = ''
                echo "HurdyGurdy Development Environment"
            '';

            packages = with pkgs; [
                gnumake
                gcc
                shaderc
                sdl3
                vulkan-validation-layers
            ];

            LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath [
                pkgs.vulkan-loader
            ];
        };
    };
}
