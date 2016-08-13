.\release\modelconverter -input data\models\sponza.dae -output data\models\sponza.gek -mode:model -flipCoords -generateNormals -smoothNormals:75 -fixMaxCoords
.\release\modelconverter -input data\models\sponza.dae -output data\models\sponza.bin -mode:tree -fixMaxCoords

.\release\modelconverter -input data\models\half_sponza.dae -output data\models\half_sponza.gek -mode:model -flipCoords -generateNormals -smoothNormals:75 -fixMaxCoords
.\release\modelconverter -input data\models\half_sponza.dae -output data\models\half_sponza.bin -mode:tree -fixMaxCoords

pause