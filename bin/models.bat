.\release\createmodel -input data\models\sponza.fbx -output data\models\sponza.gek -flipCoords -smoothAngle:75
pause

.\release\createtree -input data\models\sponza.fbx -output data\models\sponza.bin
pause

.\release\createmodel -input data\models\cube.fbx -output data\models\cube.gek -flipCoords
pause

.\release\createhull -input data\models\cube.fbx -output data\models\cube.bin
pause
