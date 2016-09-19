fn exportLight entity data =
(
	result = "<entity name=\"" + entity.name + "\">\r\n"

	position = entity.transform.pos * units.SystemScale
	rotation = entity.transform.rotation
	append result ("    <transform position=\"(" +
							(position.x as string) + ", " +
							(position.y as string) + ", " +
							(position.z as string) + ")\" rotation=\"(" +
							(rotation.x as string) + ", " +
							(rotation.y as string) + ", " +
							(rotation.z as string) + ", " +
							(rotation.w as string) + ")\" />\r\n")
	
	append result data

	color = ((entity.rgb * entity.multiplier) / 255.0)
	append result ("    <color>(" +
							(color.r as string) + ", " +
							(color.g as string) + ", " +
							(color.b as string) + ")</color>\r\n")

	append result "</entity>\r\n"

	return result
)

entityList = ""
nodeList = $* as array
for entity in nodeList do
(
	if classof(entity) == OmniLight then
	(
		data = "    <point_light range=\"" + ((entity.farAttenEnd * units.SystemScale) as string) + "\" />\r\n"
		append entityList (exportLight entity data)
	)
	else if classof(entity) == FreeSpot or classof(entity) == TargetSpot then
	(
		innerAngle = entity.hotSpot
		outerAngle = entity.falloff
		data = "    <spot_light range=\"" + ((entity.farAttenEnd * units.SystemScale) as string) +
						"\" inner_angle=\"" + (innerAngle as string) +
						"\" outer_angle=\"" + (outerAngle as string) + "\" />\r\n"
		append entityList (exportLight entity data)
	)
	else if classof(entity) == TargetSpot then
	(
		innerAngle = entity.hotSpot
		outerAngle = entity.falloff
		data = "    <spot_light range=\"" + ((entity.farAttenEnd * units.SystemScale) as string) +
						"\" inner_angle=\"" + (innerAngle as string) +
						"\" outer_angle=\"" + (outerAngle as string) + "\" />\r\n"
		append entityList (exportLight entity data)
	)
	else if classof(entity) == DirectionalLight then
	(
		data = "    <directional_light />\r\n"
		append entityList (exportLight entity data)
	)
	else if classof(entity) == TargetDirectionalLight then
	(
		data = "    <directional_light />\r\n"
		append entityList (exportLight entity data)
	)
)

print entityList
