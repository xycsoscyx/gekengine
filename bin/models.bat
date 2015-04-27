.\debug\optimizer.debug -input data\models\sponza.dae -output data\models\sponza.gek -smooth 75

.\debug\optimizer.debug -input data\models\boulder.dae -output data\models\boulder.gek -smooth 75

For /R "data/models/dungeon" %%# in (*.dae) Do (
    del "%%~dpn#.gek"
    debug\optimizer.debug -input "%%~#" -output "%%~dpn#.gek" -smooth 75
)

pause
