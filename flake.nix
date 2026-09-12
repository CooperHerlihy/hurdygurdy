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

            debug = pkgs.writeShellScriptBin "debug" ''
                cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug \
                && cmake --build build \
                && ./build/tests
            '';
            release = pkgs.writeShellScriptBin "release" ''
                cmake -G Ninja -B build/release -DCMAKE_BUILD_TYPE=Release \
                && cmake --build build/release \
                && ./build/release/tests
            '';
            san = pkgs.writeShellScriptBin "san" ''
                cmake -G Ninja -B build/san -DCMAKE_BUILD_TYPE=Debug \
                    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
                    -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
                    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
                && cmake --build build/san \
                && LSAN_OPTIONS=detect_leaks=0 ./build/san/tests
            '';
            tsan = pkgs.writeShellScriptBin "tsan" ''
                cmake -G Ninja -B build/tsan -DCMAKE_BUILD_TYPE=Debug \
                    -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer" \
                    -DCMAKE_C_FLAGS="-fsanitize=thread -fno-omit-frame-pointer" \
                    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread" \
                && cmake --build build/tsan \
                && TSAN_OPTIONS=suppressions=/dev/null ./build/tsan/tests
            '';
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
                    sdl3
                    libx11
                    libxrandr
                    pipewire
                    libxkbcommon
                    libevdev
                    wayland
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
                rev = "934c6a5f5ef2355d6df25395d555cb71f790c4e9";
                hash = "sha256-7lxmvUQvEDjYlSRsTxk99QTscO4EhS3aDGoIRrFeHyI=";
            };

            sdl3-src = pkgs.fetchFromGitHub {
                owner = "libsdl-org";
                repo = "SDL";
                rev = "release-3.4.10";
                hash = "sha256-6Dph2eLiJUmpQzPWe8EuY5LrWhrFwde2f2dwfgCcWNw=";
            };

            xorgproto-src = pkgs.fetchFromGitLab {
                domain = "gitlab.freedesktop.org";
                owner = "xorg/proto";
                repo = "xorgproto";
                rev = "fcb7e9a1a0b593a44740d83b0babddd331fea830";
                hash = "sha256-MLgV7v2ATgsbS7gVlF7R5I1r0bQ/AvUWU7/u/4BfF5c=";
            };

            libX11-src = pkgs.fetchFromGitLab {
                domain = "gitlab.freedesktop.org";
                owner = "xorg/lib";
                repo = "libX11";
                rev = "13f9b8de400335f4b86bb0672da02f8166c0e796";
                hash = "sha256-BkRNDXykZ+4bIzi1HJ5kz5E5ufCq4Jq+k2/6JvDaiHY=";
            };

            libXrandr-src = pkgs.fetchFromGitLab {
                domain = "gitlab.freedesktop.org";
                owner = "xorg/lib";
                repo = "libXrandr";
                rev = "8a4ba1974bcf07057cf891128d86ae1ab5303574";
                hash = "sha256-lyofwRfpWrAvlvWwV+uzRs3g9ZepicGQi9sdCCI75pk=";
            };

            libXrender-src = pkgs.fetchFromGitLab {
                domain = "gitlab.freedesktop.org";
                owner = "xorg/lib";
                repo = "libXrender";
                rev = "f32afe9f877ae032c6bc9c27b17b7978b1b4c856";
                hash = "sha256-rm8Y0osVIXnVpNg826VezDFHuXjKUKjYwEbCUHqfTNI=";
            };

            pipewire-src = pkgs.fetchFromGitLab {
                domain = "gitlab.freedesktop.org";
                owner = "pipewire";
                repo = "pipewire";
                rev = "fff1bcf7e5399f3945c8ed5558870696dc23597a";
                hash = "sha256-GEoSUBYHBfEoav0gb1Jov6ti4o32XPpjunmoq+sKMKU=";
            };

            libxkbcommon-src = pkgs.fetchFromGitHub {
                owner = "xkbcommon";
                repo = "libxkbcommon";
                rev = "49c8d08f7bf3e602fde221bc1680aeb27338e523";
                hash = "sha256-R4Qw+lLLDdt7I0a7A+sgTDqKaG0bfYfEcCvNBHBxRIY=";
            };

            libevdev-src = pkgs.fetchFromGitLab {
                domain = "gitlab.freedesktop.org";
                owner = "libevdev";
                repo = "libevdev";
                rev = "294f6bc00675915c3186138e84dd35614d2a20f8";
                hash = "sha256-TMPyrJdcr6TLfQYrmT+RA1uveZXJfNxyliVEdUXBRfI=";
            };

            wayland-src = pkgs.fetchFromGitLab {
                domain = "gitlab.freedesktop.org";
                owner = "wayland";
                repo = "wayland";
                rev = "381af21cf84f13be0ca24aed756a9cded3290d49";
                hash = "sha256-HGZx4f3HM+6APICDorqUyYO8YW6x97J2yO9HlH540Lg=";
            };

            wayland-protocols-src = pkgs.fetchFromGitLab {
                domain = "gitlab.freedesktop.org";
                owner = "wayland";
                repo = "wayland-protocols";
                rev = "819004adb3ab7e46f3fa3caef05b96e20434b244";
                hash = "sha256-Fal+LAXzxouWmm+u8Dfi+G69eKsbTeMN+yMkv7/C9BQ=";
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
                    rm -rf vendor/pipewire vendor/libxkbcommon vendor/libevdev
                    rm -rf vendor/wayland vendor/wayland-protocols
                    cp -r ${vulkan-headers-src} vendor/Vulkan-Headers
                    cp -r ${imgui-src} vendor/imgui
                    cp -r ${sdl3-src} vendor/SDL
                    cp -r ${xorgproto-src} vendor/xorgproto
                    cp -r ${libX11-src} vendor/libX11
                    cp -r ${libXrandr-src} vendor/libXrandr
                    cp -r ${libXrender-src} vendor/libXrender
                    cp -r ${pipewire-src} vendor/pipewire
                    cp -r ${libxkbcommon-src} vendor/libxkbcommon
                    cp -r ${libevdev-src} vendor/libevdev
                    cp -r ${wayland-src} vendor/wayland
                    cp -r ${wayland-protocols-src} vendor/wayland-protocols
                    chmod -R u+w vendor/Vulkan-Headers
                    chmod -R u+w vendor/imgui
                    chmod -R u+w vendor/SDL
                    chmod -R u+w vendor/xorgproto
                    chmod -R u+w vendor/libX11
                    chmod -R u+w vendor/libXrandr
                    chmod -R u+w vendor/libXrender
                    chmod -R u+w vendor/pipewire
                    chmod -R u+w vendor/libxkbcommon
                    chmod -R u+w vendor/libevdev
                    chmod -R u+w vendor/wayland
                    chmod -R u+w vendor/wayland-protocols
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
                        patchelf --add-rpath ${pkgs.wayland}/lib $bin
                    done
                '';
            };
        });
    };
}
