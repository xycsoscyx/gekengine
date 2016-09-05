.\release\modelconverter -input data\models\sponza.ase -output data\models\sponza.gek -mode:model -flipCoords -generateNormals -smoothNormals:75 -fixMaxCoords
pause

.\release\modelconverter -input data\models\sponza.ase -output data\models\sponza.bin -mode:tree -fixMaxCoords
pause

.\release\modelconverter -input data\models\demo.ase -output data\models\demo.gek -mode:model -flipCoords -generateNormals -smoothNormals:75 -fixMaxCoords
pause

.\release\modelconverter -input data\models\demo.ase -output data\models\demo.bin -mode:tree -fixMaxCoords
pause