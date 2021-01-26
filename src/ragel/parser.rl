#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

%%{
    machine nssrole;

    action buffer { tok = p; /* Local token start */ }

    # Discard input up to the newline
    dnl = [^\n]* '\n';

    main := |*
        space*; # Discard spaces and empty lines
        '#' dnl;
        dnl;
    *|;
}

%% write data noerror nofinal
