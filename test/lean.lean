-- Lean 4 샘플: UTF-8 토큰 스트레스
-- 1바이트 / 2바이트(α) / 3바이트(ℕ → ₁) / 4바이트(𝕜) 를 모두 포함한다

namespace Test

def double (n : ℕ) : ℕ := 2 * n

theorem double_eq (n : ℕ) : double n = n + n := by
  simp
  rfl

def compose {α β γ : Type} (f : β → γ) (g : α → β) : α → γ :=
  fun x => f (g x)

example : ∀ n : ℕ, n ≤ n + 1 := by
  intro n
  simp

def pairs : List (ℕ × ℕ) := [⟨1, 2⟩, ⟨3, 4⟩]

theorem subset_trans {s t u : Set ℕ} (h₁ : s ⊆ t) (h₂ : t ⊆ u) : s ⊆ u :=
  fun x hx => h₂ (h₁ hx)

def 𝕜 : Type := Prop

def sum₁₀ : ℕ := 55

theorem and_comm' (p q : Prop) : p ∧ q → q ∧ p :=
  fun h => ⟨h.2, h.1⟩

def label : String := "α → β with ⟨escape⟩ and \"quote\" and \\ backslash"

def ch : Char := 'λ'

def arrows : Prop := ∀ x, x ∈ ∅ → x ≠ x ∨ x ≤ x

-- maximal munch 스트레스: : vs := vs ::
def tricky (xs : List ℕ) : List ℕ := 1 :: 2 :: xs

end Test
