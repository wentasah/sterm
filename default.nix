{
  nixpkgs ? <nixpkgs>,
  pkgs ? import nixpkgs {}
}:
with pkgs;
stdenv.mkDerivation {
  name = "sterm";
  src = ./.;
  nativeBuildInputs = [ pkgs.installShellFiles ];
  installPhase = ''
    make install PREFIX=$out NO_COMP=1
    installShellCompletion --bash --name sterm completion.bash
    installShellCompletion --zsh --name _sterm completion.zsh
  '';

}
