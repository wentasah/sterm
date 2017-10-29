# Bash completion file for sterm
# vim: ft=sh

_sterm() {
	local cur prev
	_init_completion || return
	COMPREPLY=()
	#cur="${COMP_WORDS[COMP_CWORD]}"
	local ops="-h --help -c -d -e -n -r -s -v"
	case "$prev" in
		-d|-r)
			# No completion for these
			;;
		-s)
			local speeds="0 50 75 110 134 150 200 300 600 1200 1800 2400 4800 9600 19200 38400 57600 115200 230400"
			COMPREPLY+=($(compgen -W "${speeds}" -- ${cur}))
			;;
		*)
			COMPREPLY+=($(compgen -W "${ops}" -- ${cur}))
			;;
	esac
}

complete -o default -F _sterm sterm
