import re
from typing import List, Dict, Optional, Tuple


class DPLLSolver:
    def __init__(self, sentences: List[str], debug: bool = False) -> None:
        self.sentences = sentences
        self.debug = debug

        letter_count = {}
        for sentence in self.sentences:
            curr_letters = list(map(lambda x: x.strip("!"), sentence.split()))
            for letter in curr_letters:
                letter_count[letter] = letter_count.get(letter, 0) + 1

        self.letters = sorted(letter_count)

    @staticmethod
    def next(letters: List[str], idx: int, clauses: List[str]) -> Tuple[int, str, str, bool]:
        """Find the next letter to assign values to.

        Args:
            letters: Letters to be assigned
            idx: Index to look ahead from
            clauses: Clauses to solve

        Returns:
            Letter most likely to solve the clauses
        """
        # Check for unit-clause
        for i in range(idx, len(letters)):
            letter = letters[i]
            if letter in clauses:
                return i, letter, "unit literal", False
            if "!" + letter in clauses:
                return i, letter, "unit literal", True

        # Check for pure-literal
        pure_count = []
        for i in range(idx, len(letters)):
            letter = letters[i]
            rel_sentences = [sen for sen in clauses if letter in sen]

            # Check if the letter appears in a consistent boolean sign
            if all([re.match(fr".*\!{letter}.*", sen) for sen in rel_sentences]):
                pure_count.append((i, len(rel_sentences), True))
            elif all([not re.match(fr".*\!{letter}.*", sen) for sen in rel_sentences]):
                pure_count.append((i, len(rel_sentences), False))

        if pure_count:
            j, _, negate = max(pure_count, key=lambda x: x[1])
            return j, letters[j], "pure literal", negate

        # Default sorting order
        return 0, letters[idx], "", False

    def _solve_assignment(
        self, letters: List[str], clauses: List[str]
    ) -> Optional[Dict[str, bool]]:
        """Solve recursively for given letters, clauses

        Args:
            letters: Letters sorted lexicographically
            clauses: Clauses to be resolved

        Returns:
            Assignment of letter values if found
        """
        if self.debug:
            for clause in clauses:
                print(clause)

        if not clauses:
            if self.debug:
                for letter in letters:
                    print(f"unbound {letter}=false")
            return {
                letter: False for letter in letters
            }  # Correctly assigned all letters

        if "" in clauses:
            if self.debug:
                print("Contradiction")
            return None

        idx = 0
        while idx < len(letters):
            i, letter, easy, negate = self.next(letters, idx, clauses)
            prop_letters = letters[:i] + letters[i + 1 :]

            for _ in range(2):
                # Preference for negation on letter
                if negate:
                    # Solve for letter = False
                    false_clauses = []
                    for clause in clauses:
                        if letter not in clause:
                            false_clauses.append(clause)
                        elif "!" + letter in clause:
                            continue
                        else:
                            false_clauses.append(
                                re.sub(r"\s+", " ", clause.replace(letter, "")).strip()
                            )

                    if self.debug:
                        print(
                            f"{'easy case: ' + easy if easy else 'hard case: guess'} {letter}=false"
                        )

                    assignments = self._solve_assignment(prop_letters, false_clauses)
                    if assignments is not None:
                        assignments[letter] = False
                        return assignments

                # Preference for no negation on letter
                else:
                    # Solve for letter = True
                    true_clauses = []
                    for clause in clauses:
                        if letter not in clause:
                            true_clauses.append(clause)
                        elif "!" + letter in clause:
                            true_clauses.append(
                                re.sub(r"\s+", " ", clause.replace("!" + letter, "")).strip()
                            )  # Remove not occurrences

                    if self.debug:
                        print(
                            f"{'easy case: ' + easy if easy else 'hard case: guess'} {letter}=true"
                        )

                    assignments = self._solve_assignment(prop_letters, true_clauses)
                    if assignments is not None:
                        assignments[letter] = True
                        return assignments

                if self.debug:
                    print("fail")
                negate = not negate

            idx += 1

        return None

    def solve(self) -> Dict[str, bool]:
        """Solve given sentences using DPLL.

        Returns:
            Assignments for letters
        """
        if self.debug:
            print("Solving CNF using DPLL")
            print("\n".join(self.sentences))
        assignments = self._solve_assignment(self.letters, self.sentences)
        if assignments is None and self.debug:
            print("NO VALID ASSIGNMENT")

        return assignments
