#compdef sterm
#autoload

_sterm_defs() {
	_arguments : \
		"--help[Output help message]" \
		"-h[Print help text]" \
		"-s[Set baudrate]" \
		"-c[Enter command mode]" \
		"-d[Make pulse on DTR]" \
		"-r[Make pulse on RTS]" \
		"-e[Ignore '~.' escape sequence]" \
		"-n[Do not switch the device to raw mode]" \
		"-v[Verbose mode]"
	_path_files
}

_sterm() {
	if (( CURRENT > 2)); then
		local prev=${words[(( CURRENT - 1))]}
		case "${prev}" in
			-d|-r)
				# No completion for these
				;;
			-s)
				_values "Baudrate" \
					"0" \
					"50" \
					"75" \
					"110" \
					"134" \
					"150" \
					"200" \
					"300" \
					"600" \
					"1200" \
					"1800" \
					"2400" \
					"4800" \
					"9600" \
					"19200" \
					"38400" \
					"57600" \
					"115200" \
					"230400"
				;;
			*)
				_sterm_defs
				;;
		esac
	else
		_sterm_defs
	fi
}

_sterm
