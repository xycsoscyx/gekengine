.\release\modelconverter -input data\models\sponza.fbx -output data\models\sponza.gek -mode:model -flipCoords -smoothAngle:75 -unitsInFoot:29
pause

.\release\modelconverter -input data\models\sponza.fbx -output data\models\sponza.bin -mode:tree -unitsInFoot:29
pause

.\release\modelconverter -input data\models\demo.obj -output data\models\demo.gek -mode:model -flipCoords -smoothAngle:75
pause

.\release\modelconverter -input data\models\demo.obj -output data\models\demo.bin -mode:tree
pause