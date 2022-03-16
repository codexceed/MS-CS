import re
from typing import List, Callable


def find_corresponding_parenthesis(s: str, idx: int) -> int:
    """Find index of closing parenthesis corresponding to given open one.

    Args:
        s: Input string
        idx: Index of opening parenthesis

    Returns:
        Index next to that of closing parenthesis
    """
    if s[idx] != "(":
        raise Exception("Invalid input.")

    balance = 1
    for i in range(idx + 1, len(s)):
        if s[i] == "(":
            balance += 1
        elif s[i] == ")":
            balance -= 1

        if balance == 0:
            return i + 1

    raise Exception("No valid closing parenthesis found.")


def tokenize(s: str) -> List[str]:
    """Generate propositional tokens output of given BNF string.

    Args:
        s: BNF string

    Returns:
        List of tokens
    """
    tokens, i = [], 0
    while i < len(s):
        c = s[i]
        if re.match(r"[A-Z\(\)!&|]", c):
            if c == "(":
                idx_close = find_corresponding_parenthesis(s, i)
                tokens.append(s[i:idx_close])
                i = idx_close
                continue
            elif re.match(r"[A-Z]", c):
                k = i + 1
                for j in range(i+1, len(s)):
                    k += 1
                    if not re.match(r"[A-Z_]", s[j]):
                        break
                tokens.append(s[i:k])
                i = k-1
            else:
                tokens.append(c)
        elif s[i: i + 3] == "<=>":
            tokens.append("<=>")
            i += 2
        elif s[i:i + 2] == "=>":
            tokens.append("=>")
            i += 1

        i += 1

    return tokens


def with_paran(func: Callable):
    def wrapper(statement: str):
        i = 0
        left, right = "", statement
        while i < len(right):
            if right[i] == "(":
                j = find_corresponding_parenthesis(right, i)
                left += f"({wrapper(right[i+1:j-1])})"
                right = right[j:]
                i = 0
            else:
                left += right[i]
                i += 1

        return func(left)

    return wrapper


def parenthesize_for_op(tokens: List[str], op: str) -> List[str]:
    """Add parenthesized tokens for given operator.

    Args:
        tokens: Tokens to parenthesize
        op: Operator to consider

    Returns:
        List of parenthesized tokens
    """
    i = 0
    while i < len(tokens) - 1:
        if tokens[i] == op:
            p_token = f"({tokens[i - 1]} {op} {tokens[i + 1]})"
            tokens[i - 1] = p_token
            tokens.pop(i + 1)
            tokens.pop(i)
            i -= 1

        i += 1

    return tokens


@with_paran
def parenthesize(statement: str) -> str:
    """Add relevant parenthesis for implicit precedence

    Args:
        statement: BNF

    Returns:
        Statement with explicit parenthesis for implicit precedence
    """
    tokens = tokenize(statement)
    i = 0

    while i < len(tokens) - 1:
        if tokens[i] == "!":
            p_token = f"(!{tokens[i + 1]})"
            tokens[i] = p_token
            tokens.pop(i + 1)

        i += 1

    tokens = parenthesize_for_op(tokens, "&")
    tokens = parenthesize_for_op(tokens, "|")
    tokens = parenthesize_for_op(tokens, "=>")
    tokens = parenthesize_for_op(tokens, "<=>")

    return " ".join(tokens)


def remove_multiple_parenthesis(statement: str) -> str:
    """Remove multiple parenthesis encapsulating the same group

    Args:
        statement: Statement to clean

    Returns:
        Statement without multiple redundant parenthesis
    """
    i = 0

    while i < len(statement):
        if statement[i] == "(":
            # Find recurring redundant parenthesis
            j = find_corresponding_parenthesis(statement, i)
            left = i + 1
            rm_left, rm_right = [i], [j]
            while True:
                if statement[left] == "(":
                    right = find_corresponding_parenthesis(statement, left)
                    if not re.match(r"^\s*$", statement[right:rm_right[-1] - 1]):  # Only redundant if there's nothing b/w this and previous paren
                        break

                    rm_left.append(left)
                    rm_right.append(right)
                elif statement[left] != " ":
                    break

                left += 1

            fixed = statement[rm_left[-1]:rm_right[-1]]
            fixed = f"({remove_multiple_parenthesis(fixed[1:-1])})"  # Handle internal parenthesis
            fixed = statement[:i] + fixed
            statement = fixed + statement[j:]  # Remove redundant parenthesis
            i = len(fixed)
        else:
            i += 1

    return statement


def remove_redundant_parenthesis(statement: str) -> str:
    """Remove redundant parenthesis

    Args:
        statement: str

    Returns:
        str
    """
    i = 0
    left, right = "", statement
    while i < len(right):
        if right[i] == "(":
            j = find_corresponding_parenthesis(right, i)
            inner = f"({remove_redundant_parenthesis(right[i+1:j-1])})"
            inner = remove_multiple_parenthesis(inner)
            inner = re.sub(r"\((!?[A-Z])\)", r"\1", inner)
            left += inner
            right = right[j:]
            i = 0
        else:
            left += right[i]
            i += 1

    return left


def resolve_token_double_neg(tokens: List[str]) -> List[str]:
    """Remove instances of double negations from tokens.

    Args:
        tokens: List of tokens

    Returns:
        Tokens without double negation
    """
    if len(tokens) < 2:
        return tokens

    out, i = [tokens[0]], 1
    while i < len(tokens):
        if tokens[i] == tokens[i - 1] == "!":
            out.pop(-1)
        else:
            out.append(tokens[i])
        i += 1

    return out


def negate_tokens(tokens: List[str]) -> List[str]:
    """Add negation tokens for all tokens in list.

    Args:
        tokens: List of tokens

    Returns:
        List of tokens with negations added.
    """
    i = 0
    while i < len(tokens):
        if re.match(r"[A-Z(]", tokens[i][0]):
            tokens = tokens[:i] + ["!"] + tokens[i:]
            i += 1

        i += 1

    return tokens


def flip_operator(tokens: List[str]) -> List[str]:
    """Flip operator values in given token list.

    Args:
        tokens: List of tokens

    Returns:
        List of tokens with operator values flipped
    """
    l_token = len(tokens)
    for i in range(l_token):
        if tokens[i] == "|":
            tokens[i] = "&"
        elif tokens[i] == "&":
            tokens[i] = "|"

    return tokens


def demorgan_resolve(statement: str) -> str:
    """Resolve given statement using DeMorgan's Law

    Args:
        statement: Prepositional statement

    Returns:
        str
    """
    statement = statement.strip()
    tokens = tokenize(statement)
    i = 0

    while i < len(tokens):
        negation = False
        if tokens[i][0] == "(":  # Case where parenthesis
            resolve_tokens = tokenize(tokens[i][1:-1])
            if tokens[i - 1] == "!":  # Case where negation over parenthesis
                negation = True
                resolve_tokens = negate_tokens(resolve_tokens)
                resolve_tokens = resolve_token_double_neg(resolve_tokens)
                resolve_tokens = flip_operator(resolve_tokens)

            resolved = "(" + demorgan_resolve(" ".join(resolve_tokens)) + ")"

            if negation:
                tokens = tokens[: i - 1] + [resolved] + tokens[i + 1 :]
                i -= 1
            else:
                tokens = tokens[:i] + [resolved] + tokens[i + 1:]

        i += 1

    return " ".join(tokens)


@with_paran
def resolve_two_way_implies(statement: str) -> str:
    """Resolve two way implications (<=>) in a BNF statement

    Args:
        statement: BNF

    Returns:
        Statement with '<=>' resolved.
    """
    tokens = statement.split("<=>")
    if len(tokens) > 2:
        left, right = resolve_two_way_implies("<=>".join(tokens[:-1])), tokens[-1]
    elif len(tokens) < 2:
        return statement
    else:
        left, right = tokens[0], tokens[1]

    return f"({left} => {right}) & ({right} => {left})"


@with_paran
def resolve_one_way_implies(statement: str) -> str:
    """Resolve one way implications (=>) in a BNF statement.

    Args:
        statement: BNF

    Returns:
        Statement with '=>' resolved.
    """
    tokens = list(map(lambda x: x.strip(), statement.split("=>")))
    if len(tokens) < 2:
        return statement
    return f"!({tokens[0]}) | {resolve_one_way_implies('=>'.join(tokens[1:]))}"


@with_paran
def distribute_ors(statement: str) -> str:
    """Distribute the 'or' (|) operator over 'and' (&)

    Args:
        statement: BNF

    Returns:
        str
    """
    tokens = tokenize(statement)
    while True:
        i = 0
        dist = False
        while i < len(tokens) - 1:
            if tokens[i] == "|":
                if re.match(r"(\(.+&.+\)|.+&.+)", tokens[i + 1]):
                    dist = True
                    left_idx = i - 1
                    left = tokens[left_idx]

                    if tokens[i - 2] == "!":
                        left = f"!{tokens[i-1]}"
                        left_idx = i - 2

                    and_tokens = tokenize(remove_redundant_parenthesis(tokens[i + 1])[1:-1])
                    and_idx = and_tokens.index("&")

                    right_a, right_b = "".join(and_tokens[:and_idx]), "".join(and_tokens[and_idx + 1:])
                    tokens[left_idx] = "(" + distribute_ors(f"{left} | {right_a}") + ")"
                    tokens[left_idx + 1] = "&"
                    tokens[left_idx + 2] = "(" + distribute_ors(f"{left} | {right_b}") + ")"
                    tokens = tokens[: left_idx + 3] + tokens[i + 2 :]
                    i = -1

                elif re.match(r"(\(.+&.+\)|.+&.+)", tokens[i - 1]):
                    dist = True
                    right_idx = i + 1
                    right = tokens[right_idx]

                    if tokens[right_idx] == "!":
                        right_idx += 1
                        right = f"!{tokens[right_idx]}"

                    and_tokens = tokenize(remove_redundant_parenthesis(tokens[i - 1])[1:-1])
                    and_idx = and_tokens.index("&")

                    left_a, left_b = "".join(and_tokens[:and_idx]), "".join(and_tokens[and_idx + 1:])
                    tokens[i - 1] = "(" + distribute_ors(f"{left_a} | {right}") + ")"
                    tokens[i] = "&"
                    tokens[i + 1] = "(" + distribute_ors(f"{left_b} | {right}") + ")"
                    tokens = tokens[:i + 2] + tokens[right_idx + 1:]
                    i = -1

            i += 1

        if not dist:
            break

    return " ".join(tokens)


def convert(assertions: List[str], debug: bool = False) -> List[str]:
    """Convert given set of BNF propositions to CNF

    Args:
        assertions: List of BNF statements
        debug: Set true to print debug messages

    Returns:
        List of CNF statements
    """
    assertions = [stmt for stmt in assertions if stmt]
    if debug:
        print("Converting BNF to CNF")
    debug_steps = [2, 3, 5, 9]

    transform_funcs = [
        parenthesize,
        remove_redundant_parenthesis,
        resolve_two_way_implies,
        resolve_one_way_implies,
        demorgan_resolve,
        remove_redundant_parenthesis,
        distribute_ors,
        remove_redundant_parenthesis,
        lambda x: re.sub(r"[()|]", "", x),
        lambda x: re.sub(r"!\s", "!", x),
    ]

    trans_stmt = []
    for stmt in assertions:
        if debug:
            print(stmt)

        for i, transform in enumerate(transform_funcs):
            stmt = transform(stmt)

            if debug and i in debug_steps:
                print(stmt)

        trans_stmt.append(stmt)

    cnfs = []
    for stmt in trans_stmt:
        if "&" in stmt:
            cnfs += stmt.split("&")
        else:
            cnfs.append(stmt)

    cnfs = list(map(lambda x: x.strip(), cnfs))

    if debug:
        print("\nFinal CNF statements")
        print("\n".join(cnfs), "\n\n")

    return cnfs
