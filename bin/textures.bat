echo off
For /R "data/textures" %%# in (*.png,*.tga,*.jpg) Do (
    Echo %%~nx# | FIND /I "colormap" 1>NUL && (
        del "%%~dpn#.dds"
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_production -rescale lo -RescaleSinc -dxt5 -Sinc
    )
    Echo %%~nx# | FIND /I "infomap" 1>NUL && (
        del "%%~dpn#.dds"
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_production -rescale lo -RescaleSinc -dxt5 -Sinc
    )
    Echo %%~nx# | FIND /I "normalmap" 1>NUL && (
        del "%%~dpn#.dds"
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_production -rescale lo -RescaleSinc -3Dc -norm -Sinc
    )
)

pause
