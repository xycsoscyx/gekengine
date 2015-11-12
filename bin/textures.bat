echo off
For /R "data/textures" %%# in (*.png,*.tga,*.jpg) Do (
    Echo %%~nx# | FIND /I "albedo" 1>NUL && (
        del "%%~dpn#.dds"
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_production -rescale lo -RescaleSinc -dxt5 -Sinc
    )
    Echo %%~nx# | FIND /I "roughness" 1>NUL && (
        del "%%~dpn#.dds"
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_production -rescale lo -RescaleSinc -dxt5 -Sinc
    )
    Echo %%~nx# | FIND /I "normal" 1>NUL && (
        del "%%~dpn#.dds"
        nvdxt -file "%%~#" -output "%%~dpn#.dds" -quality_production -rescale lo -RescaleSinc -3Dc -Sinc
    )
)

pause
