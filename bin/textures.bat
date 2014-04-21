echo off
For /R "data/textures" %%# in (*.png) Do (
    Echo %%~nx# | FIND /I "colormap" 1>NUL && (
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_highest -rescale lo -dxt5
    )
    Echo %%~nx# | FIND /I "normalmap" 1>NUL && (
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_highest -rescale lo -3Dc
    )
)

pause
