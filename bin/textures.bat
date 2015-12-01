echo off
For /R "data/textures" %%# in (*.png,*.tga,*.jpg) Do (
    Echo %%~nx# | FIND /I "albedo" 1>NUL && (
        del "%%~dpn#.dds"
        echo %%~#
        nvcompress -fast -color -alpha -bc7 -dds10 "%%~#" "%%~dpn#.dds"
    )
    Echo %%~nx# | FIND /I "roughness" 1>NUL && (
        del "%%~dpn#.dds"
        echo %%~#
        nvcompress -fast -bc4 -dds10 "%%~#" "%%~dpn#.dds"
    )
    Echo %%~nx# | FIND /I "metalness" 1>NUL && (
        del "%%~dpn#.dds"
        echo %%~#
        nvcompress -fast -bc4 -dds10 "%%~#" "%%~dpn#.dds"
    )
    Echo %%~nx# | FIND /I "normal" 1>NUL && (
        del "%%~dpn#.dds"
        echo %%~#
        nvcompress -fast -normal -bc5 -dds10 "%%~#" "%%~dpn#.dds"
    )
)

pause
