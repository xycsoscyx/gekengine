.\debug\optimizer.debug -input data\models\sphere.raw -output data\models\sphere.gek
.\debug\optimizer.debug -input data\models\crate.raw -output data\models\crate.gek
.\debug\optimizer.debug -input data\models\ice.raw -output data\models\ice.gek
.\debug\optimizer.debug -input data\models\barrel.raw -output data\models\barrel.gek

pause

.\debug\d3map.debug -input data\worlds\demo -output data\worlds\demo -scale 20
.\debug\d3map.debug -input data\worlds\q3dm1 -output data\worlds\q3dm1 -scale 20

pause