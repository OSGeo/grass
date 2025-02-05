-- Pandoc Lua filter to handle code blocks
-- Test cases
-- raster/r.sun/r.sun.html

-- Enforces markdownlint rules during Pandoc conversion
local MAX_LINE_LENGTH = 120  -- Adjust as needed for MD013

local LIST_INDENT = ""

function wrap_text(text, max_length, indent)
    -- Wrap text at max_length while preserving words and list indentation
    local lines = {}
    local new_line = indent or ""

    for word in text:gmatch("%S+") do
        if #new_line + #word + 1 > max_length then
            table.insert(lines, new_line)
            new_line = (indent or "") .. word
        else
            new_line = (new_line == "" and word) or (new_line .. " " .. word)
        end
    end
    table.insert(lines, new_line)
    return table.concat(lines, "\n")
end

function BulletList(el)
    local markdown_list = {}

    for _, item in ipairs(el.content) do
        local item_text = pandoc.utils.stringify(item)
        local wrapped_text = wrap_text(item_text, MAX_LINE_LENGTH, LIST_INDENT .. "-")
        table.insert(markdown_list, wrapped_text)
    end

    return pandoc.RawBlock("markdown", table.concat(markdown_list, "\n"))
end

function OrderedList(el)
    local markdown_list = {}

    for i, item in ipairs(el.content) do
        local item_text = pandoc.utils.stringify(item)
        local wrapped_text = wrap_text(item_text, MAX_LINE_LENGTH, LIST_INDENT .. i .. ".")
        table.insert(markdown_list, wrapped_text)
    end

    return pandoc.RawBlock("markdown", table.concat(markdown_list, "\n"))
end


function Image(el)
    -- Convert HTML <img> to Markdown ![alt text](src)
    local alt_text = el.attributes.alt or ""
    local src = el.src
    return pandoc.Para({pandoc.RawInline("markdown", "![" .. alt_text .. "](" .. src .. ")")})
end

function Header(el)
    return pandoc.Header(el.level, el.content) -- Ensure ATX-style headers
end

function CodeBlock(el)
    -- Ensure fenced code blocks with backticks
    local lang = el.classes[1] or "sh"  -- Preserve language if available
    return pandoc.RawBlock("markdown", "```" .. lang .. "\n" .. el.text .. "\n```")
end

function Str(el)
    local text = el.text:gsub("%s+$", "") -- Remove trailing spaces
    return pandoc.Str(text)
end


function Para(el)
    local text = pandoc.utils.stringify(el)
    if #text > MAX_LINE_LENGTH then
        return pandoc.Para(wrap_text(text, MAX_LINE_LENGTH))
    end
    return el
end

function Pandoc(doc)
    -- Process document with defined rules
    local new_blocks = {}
    local previous_blank = false

    for _, block in ipairs(doc.blocks) do
        if block.t == "Para" and #block.content == 0 then
            if not previous_blank then
                table.insert(new_blocks, block)
            end
            previous_blank = true
        else
            table.insert(new_blocks, block)
            previous_blank = false
        end
    end

    return pandoc.Pandoc(new_blocks)
end
