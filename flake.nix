{
    description = "HurdgGurdy dev env";

    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs/unstable";
    };

    outputs = { self, nixpkgs, ... }:
    let
        system = "x86_64-linux";
    in {
        devShells."${system}".default = let
            pkgs = import nixpkgs { inherit system };
        in pkgs.mkShell {
            packages = with pkgs; [
                make
                gcc
                shaderc
                sdl3
            ];
        };
    };
}
