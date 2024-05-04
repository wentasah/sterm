set -l baudrates 0 50 75 110 134 150 200 300 600 1200 1800 2400 4800 9600 \
    19200 38400 57600 115200 230400 460800 500000 576000 921600 \
    1000000 1152000 1500000 2000000 2500000 3000000 3500000 4000000

complete -c sterm -s b -rf -d 'Send break signal'
complete -c sterm -s c -d 'Enter command mode'
complete -c sterm -s d -f -d 'Make pulse on DTR'
complete -c sterm -s e -d 'Ignore \'~.\' escape sequence'
complete -c sterm -s h -l help -d 'Print help and exit'
complete -c sterm -s n -d 'Do not switch stdin TTY to raw mode'
complete -c sterm -s r -f -d 'Make pulse on RTS'
complete -c sterm -s s -rfa "$baudrates" -d 'Set serial line baudrate'
complete -c sterm -s t -rf -d 'Minimum delay between two transmitted characters'
complete -c sterm -s v -f -d 'Verbose mode'
complete -c sterm -s V -l version -d 'Print version and exit'
