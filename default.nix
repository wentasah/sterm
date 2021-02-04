{
  nixpkgs ? <nixpkgs>,
  pkgs ? import nixpkgs {}
}:
with pkgs;
stdenv.mkDerivation {
  name = "sterm";
  src = ./.;
  installPhase = "make install PREFIX=$out";
}
