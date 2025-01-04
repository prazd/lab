function greetings(...)
     local result = "Hello"
     for i, v in ipairs{...} do
          result = result .. " " .. v .. ","
     end
     return result
end     