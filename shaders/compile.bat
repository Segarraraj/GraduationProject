@ECHO OFF
FOR /r %%v IN (*.hlsl) DO (
	"../tools/dxc_2022_12_16/bin/x64/dxc.exe" -T lib_6_3 -Fo %%~nv.dxil %%v
	ECHO %%~nv compiled
)
PAUSE
