open class Lexer [
    str: source;
    i: pos;
    i: line;
    i: column;

    create c peek() [
        let s: str, inst.source;
        if inst.pos < #{s} [
            out s#inst.pos;
        ] else [
            out '$$0';
        ]
    ]

    create c advance() [
        let s: str, inst.source;
        let char: c, s#inst.pos;
        inst.column ~ inst.column + 1;
        inst.pos ~ inst.pos + 1;
        out char;
    ]
]

let lex: Lexer, ?Lexer("hi", 0, 1, 0);
`(call lex.advance());
`(call lex.advance());
`(lex.pos);
`(lex.column);
