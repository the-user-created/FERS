# Contributing to FERS

Thank you for your interest in contributing to the FERS project! We welcome contributions from the community to help
improve the codebase, documentation, testing, and more. To ensure a smooth contribution process, please follow the
guidelines below.

## Table of Contents

- [Getting Started](#getting-started)
- [Reporting Issues](#reporting-issues)
- [Contributing Code](#contributing-code)
  - [Code Style Guidelines](#code-style-guidelines)
  - [Commit Message Guidelines](#commit-message-guidelines)
  - [Pull Request Process](#pull-request-process)
- [Testing](#testing)
- [Documentation Contributions](#documentation-contributions)
- [Additional Notes](#additional-notes)

## Getting Started

To get started with contributing to FERS:

1. [Fork the repository](#fork-the-repository): This allows you to freely experiment with changes in your own copy of
   the project.
2. [Clone the repository](#clone-the-repository): Clone the repository to your local machine.
    ```bash
    git clone --recursive https://github.com/the-user-created/FERS.git
    ```
3. [Set up the development environment](#set-up-the-development-environment): Install the required dependencies and
   build the project. See [README.md](README.md) for detailed instructions.
4. [Create a new branch](#create-a-new-branch): Create a new branch for your contribution.
    ```bash
    git checkout -b your-branch-name
    ```

## Reporting Issues

If you find a bug, experience an issue, or have a feature request, please report it using the GitHub Issues feature.
When submitting an issue, please include the following information:

- Clear description of the problem.
- Steps to reproduce the issue.
- Expected and actual behavior.
- Any relevant log output or screenshots.

Before submitting an issue, please:

- Search existing issues to avoid duplicates.
- Ensure your issue has not already been fixed in the latest version of the repository.

## Contributing Code

We encourage contributions to the codebase, whether it's fixing a bug, adding a new feature, or improving existing
functionality. Before contributing code, make sure to adhere to the following guidelines.

### Code Style Guidelines

- **C++ Standards**: Use modern C++ standards (C++20/23). Use features such as smart pointers, lambda expressions,
  structured bindings, and concepts where appropriate.
- **Indentation**: Use 4 spaces for indentation.
- **Naming Conventions**:
  - Variables and functions: Use `snake_case`
  - Private class members: Use `_snake_case`
  - Classes and structs: Use `UpperCamelCase`
  - Class member functions and parameters: Use `lowerCamelCase`
  - Constants: Use `ALL_UPPER`
- **Comments**: Comment code clearly and concisely, especially in complex sections. Use Doxygen-style comments for
  functions and classes.
- **Consistency**: Ensure that the code you write follows the existing style and structure of the codebase.

### Commit Message Guidelines

- Use meaningful, descriptive commit messages that explain the "why" of the changes, not just the "what."
- Use the following types of commit messages:
  - `feat`: A new feature added to the codebase.
  - `fix`: A bug fix.
  - `chore`: Routine tasks (e.g., updating dependencies, config changes).
  - `refactor`: Refactoring code without changing external behavior.
  - `docs`: Changes to documentation (README, inline docs, etc.).
  - `style`: Changes related to formatting (spaces, semicolons, etc.).
  - `test`: Adding or updating tests.
  - `perf`: Code changes to improve performance.
  - `build`: Changes affecting the build system (e.g., webpack, npm).
  - `ci`: Continuous Integration changes (e.g., workflow updates).
  - `revert`: Reverts a previous commit.
- Start your commit message with a present-tense verb (e.g., “Fix bug in simulation,” “Add multithreading support”).
- Include relevant issue numbers in the commit message, if applicable (e.g., Fixes #123).

Example commit message:

```text
feat: add multithreading to signal processing module

This commit improves the performance of the signal processing module
by introducing a global thread pool for parallel execution. This change
results in a 2x speedup during intensive simulations.

Fixes #42
```

### Pull Request Process

1. **Test your changes**: Ensure all tests pass locally before submitting a pull request.
2. **Open a pull request**:
    - Provide a clear and descriptive title.
    - Describe your changes in detail and include the reasoning behind them.
    - Reference any issues or features your pull request is related to.
3. **Address feedback**: Be prepared to make changes based on feedback from the maintainers.
4. **Ensure tests pass**: The Continuous Integration (CI) build must pass before a pull request can be merged.

## Testing

We require that all code contributions include tests to ensure that no existing functionality is broken and that new
functionality works as expected.

- **Regression Testing Suite**: Ensure your code passes the regression testing suite, located in `test/sim_tests/`.
  - For detailed instructions on running the regression tests, refer to the [README.md](README.md) file.
  - If your changes are intended to affect the expected output of the tests, you will need to update the
    `expected_output` folders accordingly.
- **Write New Tests**: If you add new functionality, add corresponding tests in the test/ directory to verify the new
  behavior.
- **CI Integration**: All pull requests are automatically tested via CI. Ensure your changes pass the CI build before
  merging.

## Documentation Contributions

Contributions to the documentation are just as important as contributions to the code. If you notice any outdated
information, errors, or missing sections, please help us improve the documentation!

- **Code Documentation**: Use Doxygen-style comments to document code. If your changes modify existing functions or
  classes, ensure the comments reflect those changes.
- **User Documentation**: If you add a feature that affects user behavior, make sure to update the relevant sections of
  the [README.md](README.md) or other user-facing documentation.

To generate the Doxygen documentation, use:

```bash
doxygen Doxyfile
```

## Additional Notes

- **Feature Requests**: If you're considering adding a large feature, please discuss it with the maintainers first by
  opening an issue or starting a GitHub Discussion to avoid duplicating efforts or working on something out of scope.
- **License**: By contributing to FERS, you agree that your contributions will be licensed under the GPLv3 license. See
  the [LICENSE](LICENSE) file for more details.

---

Thank you for contributing to FERS! Your efforts are greatly appreciated.