class CreateDbConfigs < ActiveRecord::Migration[4.2]
  def change
    create_table :db_configs do |t|
      t.string :key
      t.string :value

      t.timestamps
    end
  end
end
