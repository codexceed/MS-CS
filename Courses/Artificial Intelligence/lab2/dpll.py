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
    def next(letters: List[str], idx: int, clauses: List[str]) -> Tuple[int, str, str]:
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
            if letter == clauses:
                return i, letter, "unit literal"

        # Check for pure-literal
        for i in range(idx, len(letters)):
            letter = letters[i]
            rel_sentences = [sen for sen in clauses if letter in sen]

            # Check if the letter appears in a consistent boolean sign
            if all([re.match(fr".*\!{letter}.*", sen) for sen in rel_sentences]) or all(
                [not re.match(fr".*\!{letter}.*", sen) for sen in rel_sentences]
            ):
                return i, letter, "pure literal"

        # Default sorting order
        return 0, letters[0], ""

    def _solve_assignment(
        self, letters: List[str], clauses: List[str]
    ) -> Optional[Dict[str, bool]]:
        """Solve recursively for given letter, clauses

        Args:
            letters: Letters sorted by frequency of occurrence
            clauses: Clauses to be resolved

        Returns:
            Assignment of letter values if found
        """
        for clause in clauses:
            print(clause)

        if not clauses:
            return {
                letter: False for letter in letters
            }  # Correctly assigned all letters

        if "" in clauses:
            if self.debug:
                print("Contradiction")
            return None

        idx = 0
        while idx < len(letters):
            i, letter, easy = self.next(letters, idx, clauses)
            prop_letters = letters[:i] + letters[i + 1 :]

            # Solve for letter = False
            false_clauses = []
            for clause in clauses:
                if letter not in clause:
                    false_clauses.append(clause)
                elif "!" + letter in clause:
                    continue
                else:
                    false_clauses.append(
                        clause.replace(letter, "").replace("  ", " ").strip()
                    )

            if self.debug:
                print(
                    f"{'easy case: ' + easy if easy else 'hard case: guess'} {letter} = false"
                )

            assignments = self._solve_assignment(prop_letters, false_clauses)
            if assignments is not None:
                assignments[letter] = False
                return assignments

            print("fail", sep="|")

            # Solve for letter = True
            true_clauses = []
            for clause in clauses:
                if letter not in clause:
                    true_clauses.append(clause)
                elif "!" + letter in clause:
                    true_clauses.append(
                        clause.replace("!" + letter, "").replace("  ", "").strip()
                    )  # Remove not occurrences

            if self.debug:
                print(
                    f"{'easy case: ' + easy if easy else 'hard case: guess'} {letter} = true"
                )

            assignments = self._solve_assignment(prop_letters, true_clauses)
            if assignments is not None:
                assignments[letter] = True
                return assignments

            idx += 1

        return None

    def solve(self) -> Dict[str, bool]:
        """Solve given sentences using DPLL.

        Returns:
            Assignments for letters
        """
        return self._solve_assignment(self.letters, self.sentences)
