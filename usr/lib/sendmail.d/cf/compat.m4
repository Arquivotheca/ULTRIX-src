############################################################
############################################################
#####
#####		Provide Backward Compatibility
#####
#####		@(#)compat.m4	4.1	(ULTRIX)	7/2/90
#####
############################################################
############################################################

#####################################################
#  General code to convert back to old style names  #
#####################################################
S5

R$+<@$-.LOCAL>		$2:$1				u@h.LOCAL => h:u
R$+<@$-.CC>		$2:$1				u@h.CC => h:u
R$+<@$=Z>		$@$2:$1				u@bhost => h:u
R$+<@$=C>		$@$2:$1				u@cchost => h:u
R$+<@$-.UUCP>		$2!$1				u@host.UUCP => host!u
R$+@$+.ARPA		$1@$2				u@host.ARPA => u@host
