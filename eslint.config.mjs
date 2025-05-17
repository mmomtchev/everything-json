import { defineConfig } from "eslint/config";
import mocha from "eslint-plugin-mocha";
import globals from "globals";
import typescriptEslint from "@typescript-eslint/eslint-plugin";
import tsParser from "@typescript-eslint/parser";
import path from "node:path";
import { fileURLToPath } from "node:url";
import js from "@eslint/js";
import { FlatCompat } from "@eslint/eslintrc";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const compat = new FlatCompat({
    baseDirectory: __dirname,
    recommendedConfig: js.configs.recommended,
    allConfig: js.configs.all
});

export default defineConfig([{
    extends: compat.extends("eslint:recommended"),

    plugins: {
        mocha,
    },

    languageOptions: {
        globals: {
            ...globals.mocha,
            ...globals.node,
        },

        ecmaVersion: 2020,
        sourceType: "commonjs",
    },

    rules: {
        semi: [2, "always"],
        quotes: ["error", "single"],
        "mocha/no-exclusive-tests": "error",
        "mocha/no-identical-title": "error",
        "mocha/no-nested-tests": "error",
    },
}, {
    files: ["test/*.ts"],

    extends: compat.extends(
        "eslint:recommended",
        "plugin:@typescript-eslint/eslint-recommended",
        "plugin:@typescript-eslint/recommended",
    ),

    plugins: {
        "@typescript-eslint": typescriptEslint,
    },

    languageOptions: {
        parser: tsParser,
    },

    rules: {
        "@typescript-eslint/ban-ts-comment": "off",
        "@typescript-eslint/no-explicit-any": "off",
    },
}, {
    files: ["**/*.mjs"],

    languageOptions: {
        ecmaVersion: 5,
        sourceType: "module",
    },
}]);