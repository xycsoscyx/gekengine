pExportDialog = newRolloutFloater "GEK Exporter" 405 215

persistent global strFileName, strBaseTextures

struct Vertex (position, texcoord)

fn GetWithoutBaseTextures strInput =
(
	strInput = substring strInput 1 (strInput.count - 4)
	if (findstring strInput strBaseTextures) == 1 then
	(
		return (substring strInput (strBaseTextures.count + 1) -1)
	)

	return strInput	
)

rollout pExportRollout "GEK Exporter" width:400 height:200
(
	button pGetFileName "Save As..." pos:[305,20] width:75 height:25
	editText pFileName "" pos:[10,20] width:275 height:25
	editText pBaseTextures "" pos:[10,65] width:250 height:25
	button pGetBaseTextures "Texture Path..." pos:[280,65] width:100 height:25
	button pExport "Export" pos:[220,155] width:100 height:25
	button pCancel "Cancel" pos:[80,155] width:100 height:25
	
	on pExportRollout open do
	(
		if strFileName == undefined then
		(
			strFileName = ""
		)
		
		if strBaseTextures == undefined then
		(
			strBaseTextures = ""
		)
		
		if strFileName != undefined then pFileName.text = strFileName
		if strBaseTextures != undefined then pBaseTextures.text = strBaseTextures
	)
	
	on pGetFileName pressed do
	(
		strFileName = getSaveFileName caption:"Save As" filename:strFileName
		if strFileName != undefined then
		(
			pFileName.text = strFileName
		)
	)
	
	on pGetBaseTextures pressed do
	(
		strBaseTextures = getSavePath caption:"Base Path" initialDir:strBaseTextures
		if strBaseTextures != undefined then
		(
			if (substring strBaseTextures strBaseTextures.count 1) != "\\" then
			(
				strBaseTextures += "\\"
			)

			pBaseTextures.text = strBaseTextures
		)
	)
	
	on pCancel pressed do
	(
		closeRolloutFloater pExportDialog
	)

	on pExport pressed do
	(
		if pFileName == undefined or pFileName.text == undefined or pFileName.text == "" then
		(
			Messagebox "No output filename specified"
			return 0
		)
		
		if geometry.count == 0 then
		( 
			Messagebox "No triangle meshes located in scene"
			closeRolloutFloater pExportDialog
			return 0
		)
		
		strBaseTextures = ""
		if pBaseTextures != undefined and pBaseTextures.text != undefined then
		(
			strBaseTextures = pBaseTextures.text
		)
		
		strFileName = pFileName.text
		pFile = fopen strFileName "wb"
		if pFile == undefined then
		(
			Messagebox ("Unable to open " + strFileName + " for output")
			closeRolloutFloater pExportDialog
			return 0
		)
		else
		(
            writeLong pFile 1481327943
            writeLong pFile 1

			aCloud = #()
			
			minimum = [ 9999999, 9999999, 9999999]
			maximum = [-9999999,-9999999,-9999999]
			writeLong pFile geometry.count
			for pObject in geometry do
			(
				pMesh = snapShotAsMesh pObject
				if pMesh == undefined then
				(
					fclose pFile
					Messagebox ("Unable to get triangle mesh from object: " + pObject.name as string)
					closeRolloutFloater pExportDialog
					return 0
				) 

				if pObject.material == undefined then
				(
					fclose pFile
					Messagebox ("Unable to locate material in object: " + pObject.name as string)
					closeRolloutFloater pExportDialog
					return 0
				) 

				pMaterial = pObject.material
				if pMaterial.diffuseMap == undefined then
				(
					fclose pFile
					Messagebox ("Objects material requires a diffuse map: " + pObject.name as string)
					closeRolloutFloater pExportDialog
					return 0
				)
				
				if pMaterial.diffuseMap.filename == undefined then
				(
					fclose pFile
					Messagebox ("Objects diffuse map requires a bitmap filename: " + pObject.name as string)
					closeRolloutFloater pExportDialog
					return 0
				)
				
				writeString pFile (GetWithoutBaseTextures pMaterial.diffuseMap.filename)

				aVertices = #()
				for nFace = 1 to pMesh.numFaces do
				(
					kFace = GetFace pMesh nFace
					kTexFace = GetTVFace pMesh nFace
					for nVertex = 1 to 3 do
					(
						kVertex = Vertex position:(GetVert pMesh kFace[4 - nVertex]) texcoord:(GetTVert pMesh kTexFace[4 - nVertex])
						append aVertices (kVertex)

						if kVertex.position[1] < minimum[1] then minimum[1] = kVertex.position[1]
						if kVertex.position[2] < minimum[2] then minimum[2] = kVertex.position[2]
						if kVertex.position[3] < minimum[3] then minimum[3] = kVertex.position[3]
						if kVertex.position[1] > maximum[1] then maximum[1] = kVertex.position[1]
						if kVertex.position[2] > maximum[2] then maximum[2] = kVertex.position[2]
						if kVertex.position[3] > maximum[3] then maximum[3] = kVertex.position[3]
					)
				)
				
				writeLong pFile aVertices.count #unsigned
				for nIndex = 1 to aVertices.count do
				(
					kVertex = aVertices[nIndex]
					writeFloat pFile (kVertex.position[1])
					writeFloat pFile (kVertex.position[3])
					writeFloat pFile (kVertex.position[2])
					writeFloat pFile (kVertex.texcoord[1])
					writeFloat pFile (kVertex.texcoord[2])
					writeFloat pFile 1
					writeFloat pFile 0
					writeFloat pFile 0
					writeFloat pFile 0
					writeFloat pFile 1
					writeFloat pFile 0
					writeFloat pFile 0
					writeFloat pFile 0
					writeFloat pFile 1
					
					append aCloud kVertex.position
				)

				writeLong pFile aVertices.count #unsigned
				for nIndex = 1 to aVertices.count do
				(
					writeShort pFile (nIndex - 1) #unsigned
				)
			)
			
			writeLong pFile aCloud.count #unsigned
			for nIndex = 1 to aCloud.count do
			(
				position = aCloud[nIndex]
				writeFloat pFile position[1]
				writeFloat pFile position[3]
				writeFloat pFile position[2]
			)

			writeFloat pFile minimum[1]
			writeFloat pFile minimum[3]
			writeFloat pFile minimum[2]
			writeFloat pFile maximum[1]
			writeFloat pFile maximum[3]
			writeFloat pFile maximum[2]
			
			fclose pFile
		)

		closeRolloutFloater pExportDialog
	)
)

addrollout pExportRollout pExportDialog
