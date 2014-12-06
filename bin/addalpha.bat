For /R "data/textures" %%# in (*.png,*.tga,*.jpg) Do (
    Echo %%~nx# | FIND /I "infomap" 1>NUL && (
		mogrify -alpha set -channel A -evaluate set 0 "%%~#"
	)
)

pause
