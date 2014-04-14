SimpleState = {}

SimpleState["OnEnter"] = function(pEntity)
end

SimpleState["OnExit"] = function(pEntity)
end

SimpleState["OnUpdate"] = function(pEntity, nGameTime, nFrameTime)
--     if pEntity:GetState()["duration"] > 0 then
--         local nTime = ((nGameTime % pEntity:GetState()["duration"]) / pEntity:GetState()["duration"])
--         local nPattern = (math.floor(#pEntity:GetState()["pattern"] * nTime) + 1)
--         local nFactor = (tonumber(pEntity:GetState()["pattern"]:sub(nPattern, nPattern)) / 9)
--         pEntity:SetProperty("light", "color", pEntity:GetState()["color"] * nFactor)
--     end
end

SimpleState["OnEvent"] = function(pEntity, strAction, kParamA, kParamB)
end

function Initialize(pEntity, strParams)
    pEntity:SetState(SimpleState)
    local aParams = string.split(strParams, ":")
    if aParams ~= nil and aParams[1] ~= nil and aParams[2] ~= nil then
        pEntity:GetState()["duration"] = tonumber(aParams[1])
        if pEntity:GetState()["duration"] > 0 then
            pEntity:GetState()["pattern"] = aParams[2]
            pEntity:GetState()["color"] = pEntity:GetProperty("light", "color")
        end
    else
        pEntity:GetState()["duration"] = 0
    end
end
