{
  nixpkgs ? <nixpkgs>,
  pkgs ? import nixpkgs {}
}:
with pkgs;
stdenv.mkDerivation {
  name = "sterm";
  src = builtins.fetchGit { url = ./.; };
  installPhase = "make install PREFIX=$out";
}
