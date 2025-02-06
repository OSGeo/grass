-- Pandoc Lua filter to handle code blocks
-- Test cases
-- raster/r.sun/r.sun.html

-- Enforces markdownlint rules during Pandoc conversion
local MAX_LINE_LENGTH = 120  -- Adjust as needed for MD013

local LIST_INDENT = ""

function RawInline(el)
    -- Convert <em> to Markdown italics, and handle special characters
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
    -- Use native CodeBlock to prevent `{=markdown}` wrapping
    local lang = el.classes[1] or "sh"  -- Preserve language if available
    return pandoc.CodeBlock(el.text, lang)
end

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

-- function BulletList(el)
--     local markdown_list = {}

--     for _, item in ipairs(el.content) do
--         local item_text = pandoc.utils.stringify(item)
--         local wrapped_text = wrap_text(item_text, MAX_LINE_LENGTH, LIST_INDENT .. "-")
--         table.insert(markdown_list, wrapped_text)
--     end

--     return pandoc.RawBlock("markdown", table.concat(markdown_list, "\n"))
-- end

-- function OrderedList(el)
--     local markdown_list = {}

--     for i, item in ipairs(el.content) do
--         local item_text = pandoc.utils.stringify(item)
--         local wrapped_text = wrap_text(item_text, MAX_LINE_LENGTH, LIST_INDENT .. i .. ".")
--         table.insert(markdown_list, wrapped_text)
--     end

--     return pandoc.RawBlock("markdown", table.concat(markdown_list, "\n"))
-- end

function Link(el)
    -- Convert <a href> links to Markdown [text](url)
    local link_text = pandoc.utils.stringify(el.content)
    local link_url = el.target
    return pandoc.RawInline("markdown", "[" .. link_text .. "](" .. link_url .. ")")
end

-- function Image(el)
--     -- Convert HTML <img> to Markdown ![alt text](src)
--     local alt_text = el.attributes.alt or ""
--     local src = el.src
--     return pandoc.Para({pandoc.RawInline("markdown", "![" .. alt_text .. "](" .. src .. ")")})
-- end

-- function DefinitionList(el)
--     local new_blocks = {}

--     for _, item in ipairs(el.content) do
--         local term = item[1]               -- Term (<dt>)
--         local definitions = item[2]        -- Definitions (<dd>)

--         -- Split the term into separate parts: first word as header, rest as sub-item
--         if #term > 1 then
--             -- First part of the term becomes the header (###)
--             -- term[1] = term[1].src
--             table.insert(new_blocks, pandoc.Header(3, term[1], {}))

--             -- Remaining parts are combined and formatted (e.g., italicized)
--             local sub_term = {}
--             for i = 2, #term do
--                 table.insert(sub_term, term[i])
--             end
--             table.insert(new_blocks, pandoc.Para(sub_term))
--         else
--             -- If the term is simple, just use it as a header
--             table.insert(new_blocks, pandoc.Header(3, term, {}))
--         end

--         -- Process the definitions and convert to blockquote format
--         for _, def in ipairs(definitions) do
--             table.insert(new_blocks, pandoc.BlockQuote(def))
--         end
--     end

--     return new_blocks
-- end

function TableCell(el)
    -- Remove inline styles from table cells
    el.attributes = {}  -- Clear all attributes like style="text-align: center;"
    return el
end

function Table(el)
    -- Ensure table is formatted as a pipe table
    return el
end

function Str(el)
    local text = el.text:gsub("%s+$", "") -- Remove trailing spaces
    return pandoc.Str(text)
end

function Header(el)
    return pandoc.Header(el.level, el.content) -- Ensure ATX-style headers
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
