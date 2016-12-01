.\release\modelconverter -input data\models\sponza.fbx -output data\models\sponza.gek -mode:model -flipCoords -smoothAngle:75
pause

.\release\modelconverter -input data\models\sponza.fbx -output data\models\sponza.bin -mode:tree
pause

.\release\modelconverter -input data\models\cube.fbx -output data\models\cube.gek -mode:model -flipCoords
pause

.\release\modelconverter -input data\models\cube.fbx -output data\models\cube.bin -mode:hull
pause
