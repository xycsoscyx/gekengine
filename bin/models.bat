.\release\modelconverter -input data\models\sponza.obj -output data\models\sponza.gek -mode:model -flipCoords -generateNormals -smoothNormals:75 -fixMaxCoords -unitsInFoot:29
pause

.\release\modelconverter -input data\models\sponza.obj -output data\models\sponza.bin -mode:tree -fixMaxCoords -unitsInFoot:29
pause

.\release\modelconverter -input data\models\demo.obj -output data\models\demo.gek -mode:model -flipCoords -generateNormals -smoothNormals:75 -fixMaxCoords
pause

.\release\modelconverter -input data\models\demo.obj -output data\models\demo.bin -mode:tree -fixMaxCoords
pause