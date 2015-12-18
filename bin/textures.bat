echo off
For /R "data/textures" %%# in (*.png,*.tga,*.jpg) Do (

    Echo %%~nx# | FIND /I "albedo" 1>NUL && (
        release\compressor -input "%%~#" -output "%%~dpn#.dds" -format:BC7 -sRGB:out -overwrite
    )
    Echo %%~nx# | FIND /I "_a" 1>NUL && (
        release\compressor -input "%%~#" -output "%%~dpn#.dds" -format:BC7 -sRGB:out -overwrite
    )

    Echo %%~nx# | FIND /I "roughness" 1>NUL && (
        release\compressor -input "%%~#" -output "%%~dpn#.dds" -format:BC4 -overwrite
    )
    Echo %%~nx# | FIND /I "_r" 1>NUL && (
        release\compressor -input "%%~#" -output "%%~dpn#.dds" -format:BC4 -overwrite
    )

    Echo %%~nx# | FIND /I "metalness" 1>NUL && (
        release\compressor -input "%%~#" -output "%%~dpn#.dds" -format:BC4 -overwrite
    )
    Echo %%~nx# | FIND /I "_m" 1>NUL && (
        release\compressor -input "%%~#" -output "%%~dpn#.dds" -format:BC4 -overwrite
    )

    Echo %%~nx# | FIND /I "normal" 1>NUL && (
        release\compressor -input "%%~#" -output "%%~dpn#.dds" -format:BC5 -overwrite
    )
    Echo %%~nx# | FIND /I "_n" 1>NUL && (
        release\compressor -input "%%~#" -output "%%~dpn#.dds" -format:BC5 -overwrite
    )

)

pause
