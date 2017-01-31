.\debug\createlevel -input data\models\sponza.fbx -output data\models\sponza.gek -flipCoords -smoothAngle:75
pause

REM .\debug\createmodel -input data\models\sponza.fbx -output data\models\sponza.gek -flipCoords -smoothAngle:75
REM pause

.\debug\createtree -input data\models\sponza.tree.fbx -output data\models\sponza.bin
pause

.\debug\createmodel -input data\models\cube.fbx -output data\models\cube.gek -flipCoords
pause

.\debug\createhull -input data\models\cube.hull.fbx -output data\models\cube.bin
pause
