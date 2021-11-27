{
  description = "Simple serial terminal";

  # Nixpkgs / NixOS version to use.
  inputs.nixpkgs.url = "nixpkgs/nixos-21.05";

  outputs = { self, nixpkgs }:
    let

      # Generate a user-friendly version number.
      version = builtins.substring 0 8 self.lastModifiedDate;

      # System types to support.
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];

      # Helper function to generate an attrset '{ x86_64-linux = f "x86_64-linux"; ... }'.
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;

      # Nixpkgs instantiated for supported system types.
      nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; overlays = [ self.overlay ]; });

    in
      {

        # A Nixpkgs overlay.
        overlay = final: prev: {
          sterm = (import ./default.nix { pkgs = final; }).overrideAttrs (old: { name = "${old.name}-${version}"; });
        };

        defaultPackage = forAllSystems (system: (import nixpkgs {
          inherit system;
          overlays = [ self.overlay ];
        }).sterm);

      };
}
