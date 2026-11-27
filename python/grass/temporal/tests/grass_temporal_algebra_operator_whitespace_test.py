"""Whitespace in the final temporal operation of algebra operators."""

import pytest

from grass.temporal.temporal_algebra import TemporalAlgebraLexer
from grass.temporal.temporal_operator import TemporalOperatorParser
from grass.temporal.temporal_raster_base_algebra import TemporalRasterAlgebraLexer
from grass.temporal.temporal_vector_algebra import TemporalVectorAlgebraLexer


OPERATORS = [
    (TemporalAlgebraLexer, "{!:,equal,r}", "select", "T_SELECT_OPERATOR"),
    (TemporalAlgebraLexer, "{#,equal,r}", "hash", "T_HASH_OPERATOR"),
    (TemporalAlgebraLexer, "{&&,equal,|,r}", "boolean", "T_COMP_OPERATOR"),
    (TemporalRasterAlgebraLexer, "{*,equal,r}", "raster", "T_ARITH1_OPERATOR"),
    (TemporalRasterAlgebraLexer, "{+,equal,r}", "raster", "T_ARITH2_OPERATOR"),
    (TemporalVectorAlgebraLexer, "{|,equal,r}", "overlay", "T_OVERLAY_OPERATOR"),
]


def tokenize(lexer_type, expression):
    lexer = lexer_type()
    lexer.build()
    lexer.lexer.input(expression)
    return list(iter(lexer.lexer.token, None))


@pytest.mark.parametrize(("lexer_type", "operator", "optype", "token_type"), OPERATORS)
@pytest.mark.parametrize("whitespace", [" ", "  ", "\t"])
@pytest.mark.parametrize("reference", ["r", "right"])
def test_whitespace_before_temporal_operation(
    lexer_type, operator, optype, token_type, whitespace, reference
):
    """Spacing the temporal operation does not change its meaning."""
    spaced = operator.replace(",r}", f",{whitespace}{reference}{whitespace}}}")
    tokens = tokenize(lexer_type, f"D = A {spaced} B")
    assert [token.type for token in tokens] == [
        "NAME",
        "EQUALS",
        "NAME",
        token_type,
        "NAME",
    ]
    parser = TemporalOperatorParser()
    parser.parse(tokens[3].value, optype=optype)
    assert parser.relations == ["equal"]
    assert parser.temporal == "r"


@pytest.mark.parametrize(("lexer_type", "operator", "optype", "token_type"), OPERATORS)
def test_invalid_temporal_operation(lexer_type, operator, optype, token_type):
    """Whitespace must not make an unknown temporal operation valid."""
    invalid = operator.replace(",r}", ", unknown}")
    with pytest.raises(SyntaxError):
        tokenize(lexer_type, f"D = A {invalid} B")
