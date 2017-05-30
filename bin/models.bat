rem .\debug\createmodel -input data\models\rendering\buddha.obj -output data\models\buddha.gek -forcematerial demo/glass/glass_diffuse
rem .\debug\createmodel -input data\models\rendering\mitsuba.obj -output data\models\mitsuba.gek -forcematerial demo/glass/glass_diffuse
rem .\debug\createmodel -input data\models\rendering\dragon.obj -output data\models\dragon.gek -forcematerial demo/glass/glass_diffuse
rem .\debug\createmodel -input data\models\rendering\teapot.obj -output data\models\teapot.gek -forcematerial demo/glass/glass_diffuse
rem .\debug\createmodel -input data\models\rendering\testObj.obj -output data\models\testObj.gek -forcematerial demo/glass/glass_diffuse
rem pause

.\debug\createmodel -input data\models\rendering\sponza.fbx -output data\models\sponza.gek -flipCoords -smoothAngle:75 -unitsinfoot:13
pause

.\debug\createtree -input data\models\physics\sponza.fbx -output data\models\sponza.bin -unitsinfoot:13
pause

.\debug\createmodel -input data\models\rendering\cube.fbx -output data\models\cube.gek -flipCoords
pause

.\debug\createhull -input data\models\physics\cube.fbx -output data\models\cube.bin
pause
