import sys
import z3


def main():
    lines = []

    with open("10-input.txt", "r", encoding="UTF-8") as f:
        lines = f.readlines()

    if len(lines) == 0:
        print("", file=sys.stderr)

    total_presses = 0

    lines = [line.strip() for line in lines]
    for line in lines:
        desired_state, *buttons, joltages = line.split(" ")

        joltages = joltages.strip("{}")
        joltages = [int(joltage) for joltage in joltages.split(",")]

        buttons = [button.strip("()") for button in buttons]
        buttons = [[int(btn) for btn in button.split(",")] for button in buttons]
        for button_idx in range(len(buttons)):
            button = buttons[button_idx]
            button_new = [0] * len(joltages)
            for num in button:
                button_new[num] = 1

            buttons[button_idx] = button_new

        # Now, we got the input in the format, where we can solve the linear expression.
        # All the buttons and joltages are vectors now, where button[ idx ] represents
        # if the joltage on index idx changes or not.
        # Now we need to solve the linear equation (over transposed vectors)
        # a_0 * button_0^T + a_1 * button_1^T + ... + a_{n-1} * button_{n-1}^T = joltages^T
        # for sum( a_i ) smallest, but positive - we need to optimize
        # e.g for input:
        #       ( 0 )         ( 0 )         ( 0 )         ( 1 )         ( 1 )   ( 7 )
        # a_0 * ( 0 ) + a_1 * ( 1 ) + a_2 * ( 0 ) + a_3 * ( 0 ) + a_4 * ( 1 ) = ( 4 )
        #       ( 0 )         ( 0 )         ( 1 )         ( 1 )         ( 0 )   ( 5 )
        #       ( 1 )         ( 1 )         ( 0 )         ( 0 )         ( 0 )   ( 3 )
        # using the recommended solver for ILP - Z3 (SAT solver)

        opt = z3.Optimize()

        solution_vector_parts = [z3.Int(f'a_{i}') for i in range(len(buttons))]
        for a_i in solution_vector_parts:
            opt.add(a_i >= 0)

        for counter_idx in range(len(joltages)):
            parts_to_sum = [solution_vector_part for solution_vector_part, button in zip(solution_vector_parts, buttons)
                            if button[counter_idx]]
            total = sum(parts_to_sum)
            opt.add(total == joltages[counter_idx])

        opt.minimize(z3.Sum(solution_vector_parts))

        if opt.check() == z3.sat:
            model = opt.model()
            total_presses += sum(
                model[solution_vector_part].as_long() for solution_vector_part in solution_vector_parts)

    print(total_presses)


if __name__ == "__main__":
    main()
