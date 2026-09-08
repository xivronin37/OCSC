open class Player [
    i: health;

    create i takeDamage(i: amount) [
        inst.health ~ inst.health - amount;
    ]
]

let p: Player, ?Player(100);
call p.takeDamage(30);
`(p.health);