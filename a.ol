enum TokenType [
    Identifier,
    Keyword,
    Let, If, Else, While, Out, Call, Create, Open, Class, Insert, From, As, Inline, Request, Send, Decouple, Enum,
    Number,
    String,
    Character,
    Plus, Minus, Star, FSlash, BSlash, Colon, Semicolon, Comma, LBracket, RBracket, LParen, RParen, LBrace, RBrace, Tilde,
    Equal, EqualEqual, NotEqual, LessThan, GreaterThan, LessThanOrEqual, GreaterThanOrEqual, Question, Dot, Backtick, At,
    L_AND, L_OR, L_NOT, L_XOR, B_AND, B_OR, B_NOT, B_XOR,
    Punctuation,
    Array, Hash, Struct, Push, Remove, Inst,
    Map,
    Int, Bool, Float, Null, UnsignedInt, UnsignedFloat, Str, Char, Sentinel,
    Unknown,
    EndOfFile
]

let keywords: map(str, i, 20), \"let", "if", "else", "while", "out", "call", "create",
    "class", "inst", "open", "insert", "from", "as", "map", "enum,", "inline", "push", "remove", "request", "send", "decouple"\,
    \2, 3, 4, 5, 6, 7, 8, 10, 62, 9, 11, 12, 13, 63, 18, 14, 60, 61, 15, 16, 17\;

open class Token [
    i: type;
    str: value;
    i: line;
    i: column;
]


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

    create n skipWhitespace() [
        let s: str, inst.source;

        while inst.pos < #{s} && (s#inst.pos == ' ' || s#inst.pos == '$$n' || s#inst.pos == '$$t' || s#inst.pos == '$$r') [
            if s#inst.pos == '$$n' [
                inst.line ~ inst.line + 1;
                inst.column = 1;
            ] else [
                inst.column ~ inst.column + 1;
            ]
            inst.pos ~ inst.pos + 1;
        ]
    ]

    create c resolveEscape() [
        let code: c, call inst.advance();

        if code == 'n' [
            out '$$n';
        ]

        if code == 't' [
            out '$$t';
        ]

        if code == '0' [
            out '$$0';
        ]

        out code;
    ]

    create n testGlobal() [
        `(#{keywords});
    ]

    create Token nextToken() [
        let buf: {arr(64), i}, \ \;
        let char: c, call inst.peek();
        
        if char >= 'a' && char <= 'z' || char >= 'A' && char <= 'Z' || char == '_' [
            let next: c, call inst.peek();
            while next >= 'a' && next <= 'z' || next >= 'A' && next <= 'Z' || next >= '0' && next <= '9' || char == '_' [   
                next = call inst.peek();
                push(buf, call inst.advance());
            ]
        ]
    ]

]

let lex: Lexer, ?Lexer("hi", 0, 1, 0);
call lex.testGlobal();

