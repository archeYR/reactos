
EnableA20:
    /* We're booted from protected mode so it's enabled on boot.
     * A20 Line manipulation on SFI platforms needs research */

DisableA20:

/* VGA text mode is not available. Simply stub it all out */
writestr:
writechr:
writehex2:
writehex4:
writehex8:
    ret


Reboot:
    /* Triple fault the CPU */
    lidt null_ivt
    int3

Relocator16Boot:
    /* No boot sectors on SFI */
    ret

null_ivt:
    .word 0
    .word 0
