!
! BOOTSTRAP ON RP, Diagnostic Supervisor
!
SET DEF HEX
SET DEF LONG
SET REL:0
HALT
UNJAM
INIT
LOAD BOOT
D R10 0		! DEVICE CHOICE 0=HP
D R11 60000010	! 10= Diagnostic Supervisor
START 2
