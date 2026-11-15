-- Pandoc Lua filter to handle code blocks
-- Test cases
-- raster/r.sun/r.sun.html

-- Enforces markdownlint rules during Pandoc conversion
local MAX_LINE_LENGTH = 120  -- Adjust as needed for MD013

local LIST_INDENT = ""

function Image(el)
    -- Convert HTML <img> to Markdown ![alt text](src)
    local alt_text = el.alt or "image-alt"
    local src = el.src
    return pandoc.Image({pandoc.Str(alt_text)}, src)
end

-- Fixes some edge cases with raw HTML elements
function RawInline(el)
    if el.format == "html" then
        if el.text:match("<em>") then
            return pandoc.RawInline("markdown", "*")
        elseif el.text:match("</em>") then
            return pandoc.RawInline("markdown", "*")
        elseif el.text:match("<i>") then
            return pandoc.RawInline("markdown", "*")
        elseif el.text:match("</i>") then
            return pandoc.RawInline("markdown", "*")
        elseif el.text:match("&nbsp;") then
            return pandoc.RawInline("markdown", " ")
        elseif el.text:match("&lt;") then
            return pandoc.RawInline("markdown", "<")
        elseif el.text:match("&gt;") then
            return pandoc.RawInline("markdown", ">")
        end
    end
    return el
end

function CodeBlock(el)
    -- Ensure fenced code blocks with backticks
    local lang = el.classes[1] or "sh"  -- Preserve language if available
    return pandoc.RawBlock("markdown", "```" .. lang .. "\n" .. el.text .. "\n```")
end

function Header(el)
    return pandoc.Header(el.level, el.content) -- Ensure ATX-style headers
end

function Str(el)
    local text = el.text:gsub("%s+$", "") -- Remove trailing spaces
    return pandoc.Str(text)
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
