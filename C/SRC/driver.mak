!IF "$(T)"=="DOS"
complain:
	@echo>con There's no DOS version of this program.

dirs $(O)\driver.obj $(B)\driver.exe: complain
	@rem Do nothing as there's nothing to do
!ENDIF

