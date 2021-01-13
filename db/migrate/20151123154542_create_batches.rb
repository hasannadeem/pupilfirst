class CreateBatches < ActiveRecord::Migration[4.2]
  def change
    create_table :batches do |t|
      t.string :name
      t.text :description
      t.date :start_date
      t.date :end_date

      t.timestamps null: false
    end
  end
end
