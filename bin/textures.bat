echo off
For /R "data/textures" %%# in (*.png,*.tga) Do (
    Echo %%~nx# | FIND /I "colormap" 1>NUL && (
        del "%%~dpn#.dds"
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_production -dxt5
    )
    Echo %%~nx# | FIND /I "normalmap" 1>NUL && (
        del "%%~dpn#.dds"
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_production -3Dc
    )
)

pause
