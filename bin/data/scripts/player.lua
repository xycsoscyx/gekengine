SimpleState = {}

SimpleState["OnEnter"] = function(pEntity)
end

SimpleState["OnExit"] = function(pEntity)
end

SimpleState["OnUpdate"] = function(pEntity, nGameTime, nFrameTime)
    local nRotation = pEntity:GetProperty("transform", "rotation")
    local nMatrix = float4x4(nRotation)

    local nForce = float3(0.0, 0.0, 0.0)
    if pEntity:GetState()["Actions"]["forward"] ~= nil and pEntity:GetState()["Actions"]["forward"] then
        nForce = (nForce + float3(nMatrix.rz))
    end
    if pEntity:GetState()["Actions"]["backward"] ~= nil and pEntity:GetState()["Actions"]["backward"] then
        nForce = (nForce - float3(nMatrix.rz))
    end
    if pEntity:GetState()["Actions"]["strafe_left"] ~= nil and pEntity:GetState()["Actions"]["strafe_left"] then
        nForce = (nForce - float3(nMatrix.rx))
    end
    if pEntity:GetState()["Actions"]["strafe_right"] ~= nil and pEntity:GetState()["Actions"]["strafe_right"] then
        nForce = (nForce + float3(nMatrix.rx))
    end
    if pEntity:GetState()["Actions"]["rise"] ~= nil and pEntity:GetState()["Actions"]["rise"] then
        nForce = (nForce + float3(nMatrix.ry))
    end
    if pEntity:GetState()["Actions"]["fall"] ~= nil and pEntity:GetState()["Actions"]["fall"] then
        nForce = (nForce - float3(nMatrix.ry))
    end
    if pEntity:GetState()["Actions"]["height"] ~= nil then
        nForce = (nForce + (float3(nMatrix.ry) * pEntity:GetState()["Actions"]["height"]))
        pEntity:GetState()["Actions"]["height"] = nil;
    end
    if pEntity:GetState()["Actions"]["turn"] ~= nil then
        nRotation = (nRotation * quaternion(0.0, pEntity:GetState()["Actions"]["turn"].x * nFrameTime, 0.0))
        pEntity:SetProperty("transform", "rotation", nRotation)
        pEntity:GetState()["Actions"]["turn"] = nil;
    end

    if nForce:GetLength() > 0 then
        nForce = nForce * 4
        local nPosition = pEntity:GetProperty("transform", "position")
        nPosition = (nPosition + (nForce * nFrameTime))
        pEntity:SetProperty("transform", "position", nPosition)
    end
end

SimpleState["OnRender"] = function(pEntity)
    if pEntity:GetState()["GUI"] then
        pEntity:EnablePass("MainMenu")
    end
end

SimpleState["OnEvent"] = function(pEntity, strAction, kParamA, kParamB)
    if strAction == "input" then
        if kParamA == "escape" and kParamB then
            pEntity:GetState()["GUI"] = not pEntity:GetState()["GUI"]
            pEntity:CaptureMouse(not pEntity:GetState()["GUI"])
        elseif not pEntity:GetState()["GUI"] then
            pEntity:GetState()["Actions"][kParamA] = kParamB
        end
    end
end

function Initialize(pEntity, strParams)
    pEntity:SetState(SimpleState)
    pEntity:GetState()["Actions"] = {}
    pEntity:GetState()["GUI"] = false
    pEntity:CaptureMouse(not pEntity:GetState()["GUI"])
end
