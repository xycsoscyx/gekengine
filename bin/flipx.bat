For /R "data/textures" %%# in (*.png,*.tga,*.jpg) Do (
    Echo %%~nx# | FIND /I "normalmap" 1>NUL && (
		mogrify -channel red -negate +channel "%%~#"
	)
)

pause
