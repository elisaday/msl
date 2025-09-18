
local num = 8
local count = 0

function search(a, s)
    a[s] = 0
    while a[s] < num do
        local ok = 1
        local x = 0
        while x < s do
            if a[x] == a[s] or math.abs(a[x] - a[s]) == (s - x) then
                ok = nil
            end

            x = x + 1
        end

        if ok ~= nil then
            if s == num - 1 then
                count = count + 1
            else
                search(a, s + 1)
            end
        end

        a[s] = a[s] + 1
    end
end

a = {0, 0, 0, 0, 0, 0, 0, 0}
t = os.clock()
search(a, 0)
print(os.clock() - t)



                    
