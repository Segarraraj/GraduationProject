@ECHO OFF
FOR /r %%v IN (*.vert.hlsl) DO (
	"../tools/dxc_2022_12_16/bin/x64/dxc.exe" -T vs_6_3 -Fo %%~nv.cso %%v
	ECHO %%~nv compiled
)

FOR /r %%v IN (*.frag.hlsl) DO (
	"../tools/dxc_2022_12_16/bin/x64/dxc.exe" -T ps_6_3 -Fo %%~nv.cso %%v
	ECHO %%~nv compiled
)
PAUSE
