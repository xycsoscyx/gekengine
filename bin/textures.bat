echo off
For /R "data/textures" %%# in (*.png,*.tga,*.jpg) Do (
    Echo %%~nx# | FIND /I "colormap" 1>NUL && (
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_production -rescale lo -RescaleSinc -dxt5 -Sinc -overwrite
    )
    Echo %%~nx# | FIND /I "infomap" 1>NUL && (
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_production -rescale lo -RescaleSinc -dxt5 -Sinc -overwrite
    )
    Echo %%~nx# | FIND /I "normalmap" 1>NUL && (
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_production -rescale lo -RescaleSinc -3Dc -Sinc -overwrite
    )
)

pause
