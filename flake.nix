{
  description = "Tiwut-ENV Minimal Wayland Compositor";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }: let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };
  in {
    devShells.${system}.default = pkgs.mkShell {
      buildInputs = with pkgs; [
        cmake
        pkg-config
        wlroots
        wayland
        wayland-protocols
        wayland-scanner
        libxkbcommon
        libdrm
        libinput
        mesa
        pixman
        gcc
        gnumake
      ];
    };
  };
}
