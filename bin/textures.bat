echo off
For /R "data/textures" %%# in (*.png,*.tga) Do (
    Echo %%~nx# | FIND /I "colormap" 1>NUL && (
        del "%%~dpn#.dds"
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_production -rescale lo -dxt5
    )
    Echo %%~nx# | FIND /I "infomap" 1>NUL && (
        del "%%~dpn#.dds"
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_production -rescale lo -dxt5
    )
    Echo %%~nx# | FIND /I "normalmap" 1>NUL && (
        del "%%~dpn#.dds"
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_production -rescale lo -3Dc
    )
)

pause
