-- Pandoc Lua filter to handle code blocks
-- Test cases
-- raster/r.sun/r.sun.html

-- Function to convert code blocks to markdown
function CodeBlock (cb)
    return pandoc.RawBlock('markdown', '```bash\n' .. cb.text .. '\n```\n')
end
