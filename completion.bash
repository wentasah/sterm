# Bash completion file for sterm
# vim: ft=sh

_sterm() {
	local cur prev
	_init_completion || return
	COMPREPLY=()
	#cur="${COMP_WORDS[COMP_CWORD]}"
	local ops="-h --help -c -d -e -n -r -s -v"
	case "$prev" in
		-b|-d|-r)
			# No completion for these
			;;
		-s)
			local speeds="0 50 75 110 134 150 200 300 600 1200 1800 2400 4800 9600 19200 38400 57600 115200 230400
				460800 500000 576000 921600 1000000 1152000 1500000 2000000 2500000 3000000 3500000 4000000"
			COMPREPLY+=($(compgen -W "${speeds}" -- ${cur}))
			;;
		*)
			COMPREPLY+=($(compgen -W "${ops}" -- ${cur}))
			;;
	esac
}

complete -o default -F _sterm sterm
